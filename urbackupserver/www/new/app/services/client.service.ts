import {Client}   from './../models/client';
import {ClientFilterRequest} from './../models/clientFilterRequest';

export class ClientService {
	clients : Client[];
	
	constructor() {
		this.clients = [
			new Client("Pc1"),
			new Client("PC2")
		];
	}
	
	getClients(filterRequest: ClientFilterRequest) { 
		var filteredClients = [];
		
		for (var i in this.clients){
			if(this.clients[i].name.toUpperCase().search(filterRequest.name.toUpperCase()) != -1){
				filteredClients.push(this.clients[i]);
			}
		}
		
		return filteredClients; 
	}
}