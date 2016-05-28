"use strict";
var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (this && this.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};
var core_1 = require('@angular/core');
var clientSearchRequest_1 = require('./../../models/clientSearchRequest');
var client_service_1 = require('./../../services/client.service');
var ClientListComponent = (function () {
    function ClientListComponent(clientService) {
        this.clientService = clientService;
        this.searchRequest = new clientSearchRequest_1.ClientSearchRequest();
        this.search();
    }
    ClientListComponent.prototype.search = function () {
        if (this.searchRequest.online == "0: null") {
            this.searchRequest.online = null;
        }
        this.searchResult = this.clientService.getClients(this.searchRequest);
    };
    ClientListComponent.prototype.toggleClientDetail = function (event) {
        var row = $(event.currentTarget).parent();
        var detailRow = row.next();
        detailRow.toggle();
    };
    ClientListComponent.prototype.toggleClient = function (event, client) {
        client.selected = event.currentTarget.checked;
    };
    ClientListComponent.prototype.toggleAllClients = function (event) {
        for (var i in this.searchResult.clients) {
            this.searchResult.clients[i].selected = event.currentTarget.checked;
        }
    };
    ClientListComponent.prototype.getSelectedClients = function () {
        var clients = [];
        for (var i in this.searchResult.clients) {
            if (this.searchResult.clients[i].selected) {
                clients.push(this.searchResult.clients[i]);
            }
        }
        return clients;
    };
    ClientListComponent = __decorate([
        core_1.Component({
            selector: 'clientlist',
            templateUrl: './app/components/clientlist/clientlist.html',
            providers: [client_service_1.ClientService]
        }), 
        __metadata('design:paramtypes', [client_service_1.ClientService])
    ], ClientListComponent);
    return ClientListComponent;
}());
exports.ClientListComponent = ClientListComponent;
//# sourceMappingURL=clientlist.component.js.map