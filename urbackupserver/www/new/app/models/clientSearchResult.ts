import {ClientSearchRequest} from './clientSearchRequest';
import {Client} from './client';

export class ClientSearchResult {
  request: ClientSearchRequest;
  clients : Client[];
  itemCount: number;
  
	constructor(request: ClientSearchRequest, clients: Client[]) {
		this.request = request;
		this.clients = clients;
	}
}