import {Component} from 'angular2/core';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html'
})

export class ClientListComponent {  
	_clients = [{"name":"Pc1"},{"name":"PC2"}];
	
	clients = this._clients; 
	filter = {name : "",online : "",status : ""};
	
	executeFilter(){
		this.clients = [];
		
		for (var i in this._clients){
			if(this._clients[i].name.toUpperCase().search(this.filter.name.toUpperCase()) != -1){
				this.clients.push(this._clients[i]);
			}
		}
	}
	
	displayDetail(clientNameCell){
		var row = $(clientNameCell).parent();
		var detailRow = row.next();
		detailRow.show();
	}
}