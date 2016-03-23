import {Component} from 'angular2/core';

@Component({
    selector: 'clientlist',
	templateUrl: './app/components/clientlist/clientlist.html'
})

export class ClientListComponent {  
	clients = [{"Name":"Pc1"},{"Name":"PC2"}];
}