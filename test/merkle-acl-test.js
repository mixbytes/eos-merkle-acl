const binaryen = require('binaryen')
const common = require('./common');
const axios = require('axios');
const Eos = require('eosjs');
const fs = require('fs');
//const merkle = require('merkle');
//const crypto = require('crypto');
const MerkleTree = require('./merkleTree');

function makeid()
{
    var text = "";
    var possible = "abcdefghijklmnopqrstuvwxyz";

    for (var i = 0; i < 12; i++) {
        text += possible.charAt(Math.floor(Math.random() * possible.length));
    }
    return text;
}

function mockValidator(root, leaf, proof) {
    let current = leaf;

    proof.forEach((el) => {
        current = Buffer.from(Eos.modules.ecc.sha256(Buffer.concat([current, el].sort(Buffer.compare))), "hex");
    });

    console.log("current: ", current);
    console.log("root: ", root);
    return Buffer.compare(root, current);
}

common.run({ name : 'Create merkle-acl contract', version : '0.0.1' })
    .then(async (app) => {
        //const node_url = 'https://frodo.eosnode.smartz.io:18888';
        const node_url = 'http://127.0.0.1:8888';

        // [TODO] make all async normally, not like this shit
        let getInfoResp = await axios.get(node_url + '/v1/chain/get_info').catch(function(error) {
            throw ("EOS node at '" + node_url + "is not available");
        });

        // [TODO] output much more info about error, (connection, response) and move to separate function pingNode
        if (getInfoResp.status !== 200) {
            throw ('EOS node not returned 200 OK');
        }

        const eosio_wif = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3";
        const eosio_pub = Eos.modules.ecc.privateToPublic(eosio_wif);

        let eos = Eos({
            chainId : getInfoResp.data.chain_id,
            keyProvider : eosio_wif,
            httpEndpoint : node_url,
            binaryen : binaryen,
        });

        let contractUser = makeid();
        let contractWif = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3";
        let contractPub = Eos.modules.ecc.privateToPublic(contractWif);
        const account = await eos.newaccount({
            creator : "eosio",
            name : contractUser,
            owner : contractPub,
            active : contractPub,
        });

        console.log("CONTRACT: ", contractUser);

        let ceos = Eos({
            chainId : getInfoResp.data.chain_id,
            keyProvider : contractWif,
            httpEndpoint : node_url,
            binaryen : binaryen,
        });

        let wasm = fs.readFileSync('../build/merkle-acl.wast');
        if (typeof (wasm) == 'undefined') {
            throw ("Error reading .wast file");
        };

        let abi_json = fs.readFileSync('../build/merkle-acl.abi');
        if (typeof (abi_json) == 'undefined') {
            throw ("Error reading .abi file");
        };

        // [NOTE] HZ WTF?
        abi = JSON.parse(abi_json);
        // abi.version = "eosio::abi/1.0";

        try {
            await ceos.setcode(contractUser, 0, 0, wasm);
            await ceos.setabi(contractUser, abi);
        } catch (e) {
            throw ("Set code failed: " + e);
        }

        let contract = null;
        await ceos.contract(contractUser).then(c => contract = c);
        console.log("Created user '" + contractUser + "' with pubkey: " + contractPub + ", set merkle contract");

        console.log("\n\nBegin to generate users");
        let N = 10;
        let accounts_names = [];
        let accounts = [];
        for (var i = 0; i < N; i++) {
            const wif = Eos.modules.ecc.seedPrivate(Math.random().toString());
            const pub = Eos.modules.ecc.privateToPublic(wif);
            const user = makeid();
            // sorryyyy
            accounts_names.push(user);
            accounts.push({ name : user, pub : pub, priv : wif });

            const account = await eos.newaccount({
                creator : "eosio",
                name : user,
                owner : pub,
                active : pub
            });
            console.log("Created user '" + user);
        }

        let allowed_accounts = [];
        let not_allowed_accounts = [];
        for (let i = 0; i < accounts_names.length; i++) {
            if (i % 2 == 0) {
                allowed_accounts.push(accounts_names[i]);
            } else {
                not_allowed_accounts.push(accounts_names[i]);
            }
        }
        console.log("Split users, " + allowed_accounts.length + " allowed, " + not_allowed_accounts.length + " not allowed");

        // [FIXME] MAIN place - this .js module is not compatible with proofs, made in libraries/chain/merkle.cpp
        // replace with compatible one, rewrite .cpp implementation, or wait until EOS will include merkle methods
        // in WASM interface

        let merkleTree = await new MerkleTree(allowed_accounts);
        const merkleHexRoot = await merkleTree.getHexRoot().substring(2);
        const merkleRoot = merkleTree.getRoot();
        try {
            let trx = await contract.mrklsetprms({ merkle_root : merkleRoot }, { authorization : contractUser });
            console.log("Built merkle tree from allowed accounts, root: " + merkleHexRoot + " was written to contract");
        } catch (e) {
            console.log(e);
        }

        for (let i = 0; i < allowed_accounts.length; i++) {
            let a = allowed_accounts[i];
            let hh = Buffer.from(Eos.modules.ecc.sha256(a), "hex");
            let mProof = merkleTree.getProof(a);
            //console.log(a);
            //console.log(hh);
            //console.log(mProof);
            try {
        //        console.log("VALIDATE: ", mockValidator(merkleRoot, hh, mProof));
                let trx = await contract.merklemint({ from : a, acc_name_hash : hh, merkle_proof : mProof }, { authorization : contractUser });
                console.log("Account is Ok:", a);
            } catch (e) {
                console.log(e);
            }
        }

        for (let i = 0; i < not_allowed_accounts.length; i++) {
            let a = not_allowed_accounts[i];
            let hh = Buffer.from(Eos.modules.ecc.sha256(a), "hex");
            //let mProof = await merkleTree.getProof(a);
            //console.log(a);
            //console.log(hh);
            //console.log(mProof);
            try {
                //let trx = await contract.merklemint({from: a, acc_name_hash: hh, merkle_proof: mProof}, {authorization: contractUser});
            } catch (e) {
                console.log(e);
            }
        }
    })
    .catch(function(e) {
        console.log(e);
    });
