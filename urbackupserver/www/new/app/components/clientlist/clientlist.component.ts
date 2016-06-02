import {Component} from '@angular/core';
import {OnInit} from '@angular/core';
import {Client} from './../../models/client';
import {ClientStatus} from './../../models/clientStatus';
import {ClientSearchRequest} from './../../models/clientSearchRequest';
import {ClientSearchResult} from './../../models/clientSearchResult';
import {ClientService} from './../../services/client.service';
import {RelativeTimePipe} from './../../pipes/relativeTimePipe';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html',
	providers: [ClientService],
	pipes: [RelativeTimePipe],
})

export class ClientListComponent implements OnInit {  
	clientService : ClientService;
	searchRequest : ClientSearchRequest;
	searchResult: ClientSearchResult;
	pendingSearchRequest : any;
	
	constructor(clientService: ClientService) {
		this.clientService = clientService;
		this.searchRequest = new ClientSearchRequest();
	}

	ngOnInit() {
		this.search();
	}
	
	search(pageNumber: number = null) {
		if (pageNumber != null) {
			this.searchRequest.pageNumber = pageNumber;
		}

		this.searchResult = null;
		this.clientService.getClients(this.searchRequest).then(result => this.searchResult = result);
	}

	delayedSearch() {
		if (this.pendingSearchRequest != null) {
			clearTimeout(this.pendingSearchRequest);
		}

		var controller = this;

		this.pendingSearchRequest = setTimeout(() => {
			controller.search(1);
			controller.pendingSearchRequest = null;
		}, 750);
	}
	
	toggleAllClientsSelection(event: any){
		for (var i in this.searchResult.clients){
			this.searchResult.clients[i].selected = event.currentTarget.checked;	
		}
	}
		
	getSelectedClients(){		
		var clients = [];

		if (this.searchResult) {
			for (var i in this.searchResult.clients) {
				if (this.searchResult.clients[i].selected) {
					clients.push(this.searchResult.clients[i]);
				}
			}
		}

		return clients;
	}
}