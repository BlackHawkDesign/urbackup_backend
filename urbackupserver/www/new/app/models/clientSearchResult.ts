import {ClientSearchRequest} from './clientSearchRequest';
import {Client} from './client';

export class ClientSearchResult {
	request: ClientSearchRequest;
	clients: Client[];
	clientCount: number;
	pages: number[];
  
	constructor(request?: ClientSearchRequest, clients?: Client[], clientCount?: number) {
		this.request = request || new ClientSearchRequest();
		this.clients = clients || [];
		this.clientCount = clientCount || 0;
		this.generatePages();
	}

	generatePages() {
		var pageNumber = 1;
		this.pages = [];

		while (this.clientCount > ((pageNumber-1) * this.request.pageSize)) {
			this.pages.push(pageNumber);
			pageNumber++;
		}
	}
}