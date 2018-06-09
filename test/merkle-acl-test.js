const binaryen = require('binaryen')
const common = require('./common');
const axios = require('axios');
const Eos = require('eosjs');
const fs = require('fs');
//const merkle = require('merkle');
//const crypto = require('crypto');  
const MerkleTree = require('./merkleTree');


function makeid() {
  	var text = "";
  	var possible = "abcdefghijklmnopqrstuvwxyz";

  	for (var i = 0; i < 12; i++) {
    	text += possible.charAt(Math.floor(Math.random() * possible.length));
	}
  return text;
}


common.run({name: 'Create merkle-acl contract', version: '0.0.1'})
    .then(async (app) => {
		//const node_url = 'https://frodo.eosnode.smartz.io:18888';
		const node_url = 'http://127.0.0.1:8888';
		let N=10;
		let accounts_names = [];
		let accounts = [];
		for (var i = 0; i < N; i++) {
			const wif = Eos.modules.ecc.seedPrivate(Math.random().toString());
    		const pub = Eos.modules.ecc.privateToPublic(wif);
			const user = makeid();
			// sorryyyy
			accounts_names.push(user);
			accounts.push({ name: user, pub: pub, priv: wif });
	
			/*const account = await eos.newaccount({
      		creator: "eosio",
      		name: user,
      		owner: pub,
      		active: pub
    		});*/
		}

		// [TODO] make all async normally, not like this shit		
        let getInfoResp = await axios.get(node_url+'/v1/chain/get_info').catch(function (error) {
			throw("EOS node at '" + node_url + "is not available");
		 });

		// [TODO] output much more info about error, (connection, response) and move to separate function pingNode
		if (getInfoResp.status !== 200) {
            throw('EOS node not returned 200 OK');
		}

		const eosio_wif = "5KbuFQTX8zb31NNfwV87baBaJXH8Ukt2ix3MseraRZtniFCPm6W";
		const eosio_pub = Eos.modules.ecc.privateToPublic(eosio_wif);

		let eos = await Eos({
            chainId: getInfoResp.data.chain_id,
            keyProvider: eosio_wif,
            httpEndpoint: node_url,
				binaryen: binaryen,
        });

		const account = await eos.newaccount({
      		creator: "pril",
      		name: accounts[0].name,
      		owner: accounts[0].pub,
      		active: accounts[0].pub
    	});


		let wasm = await fs.readFileSync('../build/merkle-acl.wast');
		if (typeof(wasm) == 'undefined') {
			throw("Error reading .wast file");
		};

		let abi_json = await fs.readFileSync('../build/merkle-acl.abi');
		if (typeof(abi_json) == 'undefined') {
			throw("Error reading .abi file");
		};

		// [NOTE] HZ WTF?
		abi = JSON.parse(abi_json);
		// abi.version = "eosio::abi/1.0";

		let contractUser = accounts[0].name;
		let contractWif = accounts[0].priv;
		let contractPub = accounts[0].pub;

		let ceos = await Eos({
            chainId: getInfoResp.data.chain_id,
            keyProvider: contractWif,
            httpEndpoint: node_url,
				binaryen: binaryen,
      });

		try {
			await ceos.setcode(contractUser, 0, 0, wasm);
			await ceos.setabi(contractUser, abi);
		}
		catch(e) {
			throw("Set code failed: " + e);
		}
	
		let contract = null;
		await ceos.contract(contractUser).then(c => contract = c);
		await(console.log("Created user '" + contractUser + "' with pubkey: " + contractPub + ", set merkle contract"));

		let merkleTree = new MerkleTree(accounts_names);                                                                               
      const merkleRootHex = merkleTree.getHexRoot();                                                                                   
      const mProof = await merkleTree.getProof(accounts_names[1]);                                                                   
		let hh = Buffer.from(Eos.modules.ecc.sha256(accounts_names[1]), "hex");
		
		//console.log(hh);
		//console.log(mProof);

		try {                                                                                                                                                 
			let trx = await contract.merklemint(accounts_names[1], hh, mProof, {authorization: contractUser});
			console.log(trx);                                                                                                                                 
	  	}                                                                                                                                                     
	  	catch (e) {                                                                                                                                           
			console.log("Fail");                                                                                                                              
			console.log(e);                                                                                                                                   
	  	} 
	}).catch(function(e) {
		console.log(e);
	});
