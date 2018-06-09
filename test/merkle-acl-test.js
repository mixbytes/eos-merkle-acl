const binaryen = require('binaryen')
const common = require('./common');
const axios = require('axios');
const Eos = require('eosjs');
const fs = require('fs');

common.run({name: 'Create merkle-acl contract', version: '0.0.1'})
    .then(async (app) => {
		const params = {
			owner: 'merkleraaaaa',
			private_key: '5JYTCryPfL9t8Z1axFwdeJjsf9ECfECdHSZg4UeQp6EJM9dcpco',
			public_key: 'EOS4y4hQrvQp162J7sBpMAG2nckZymU5yXah7yqADrQwGAcuHBjA8',
			node_url: 'https://frodo.eosnode.smartz.io:18888'
		}

		// [TODO] make all async normally, not like this shit		
        let getInfoResp = await axios.get(params.node_url+'/v1/chain/get_info').catch(function (error) {
      		console.log("EOS node at '" + params.node_url + " is not available");
			throw("EOS node at '" + params.node_url + "is not available");
		 });

		// [TODO] output much more info about error, (connection, response) and move to separate function pingNode
		if (getInfoResp.status !== 200) {
            throw('EOS node not returned 200 OK');
		}

        let eos = await Eos({
            chainId: getInfoResp.data.chain_id,
            keyProvider: params.private_key,
            httpEndpoint: params.node_url,
			binaryen: binaryen,
        });

		let wasm = await fs.readFileSync('../build/contract.wast');
		if (typeof(wasm) == 'undefined') {
			throw("Error reading .wast file");
		};

		let abi = await fs.readFileSync('../build/contract.abi');
		if (typeof(abi) == 'undefined') {
			throw("Error reading .abi file");
		};

		// [NOTE] HZ WTF?
		abi.version = "eosio::abi/1.0";

		try {
			await eos.setcode(params.owner, 0, 0, wasm);
			await eos.setabi(params.owner, JSON.parse(abi));
		}
		catch(e) {
			throw("Set code failed: " + e);
		}

		throw("FINISHED");


		//let contract = null;
		//eos.contract(params.owner).then(c => contract = c);
		//console.log(contract);
		/*
        try {                                                                                                                                                 
            let trx = await eos.transaction('eosio', (system) => {                                                                                            
                system.delegatebw({                                                                                                                           
                    'from': params.name,                                                                                                                      
                    'receiver': params.name,                                                                                                                  
                    'stake_net_quantity': params.net,                                                                                                         
                    'stake_cpu_quantity': params.cpu,                                                                                                         
                    'transfer': 0                                                                                                                             
                });                                                                                                                                           
            });                                                                                                                                               
                                                                                                                                                              
            console.log("OK staked");                                                                                                                         
            console.log(trx);                                                                                                                                 
        }                                                                                                                                                     
        catch (e) {                                                                                                                                           
            console.log("Fail");                                                                                                                              
            console.log(e);                                                                                                                                   
        }                     
        console.log("Account created, tx: ");
        console.log(trx);
	*/
	}).catch(function(e) {
		console.log(e);
	});
