import {Client}   from './../models/client';
import {ClientSearchRequest} from './../models/clientSearchRequest';

export class ClientService {
	clients : Client[];
	
	constructor() {
		this.clients = [
			new Client("Pc1"),
			new Client("PC2")
		];
	}
	
	getClients(searchRequest: ClientSearchRequest) { 
		var clients = [];
		
		for (var i in this.clients){
			if(this.clients[i].name.toUpperCase().search(searchRequest.name.toUpperCase()) != -1){
				clients.push(this.clients[i]);
			}
		}
		
		return clients; 
	}
}