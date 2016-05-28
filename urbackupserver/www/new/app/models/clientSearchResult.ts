import {ClientSearchRequest} from './clientSearchRequest';
import {Client} from './client';

export class ClientSearchResult {
	request: ClientSearchRequest;
	clients: Client[];
	clientCount: number;
	pages: number[];
  
	constructor(request: ClientSearchRequest, clients: Client[], clientCount : number) {
		this.request = request;
		this.clients = clients;
		this.clientCount = clientCount;
		this.generatePages(request, clients);
	}

	generatePages(request: ClientSearchRequest, clients: Client[]) {
		var pageNumber = 1;
		this.pages = [];

		while (this.clientCount > ((pageNumber-1) * request.pageSize)) {
			this.pages.push(pageNumber);
			pageNumber++;
		}
	}
}