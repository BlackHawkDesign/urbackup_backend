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
var relativeTimePipe_1 = require('./../../pipes/relativeTimePipe');
var ClientListComponent = (function () {
    function ClientListComponent(clientService) {
        this.clientService = clientService;
        this.searchRequest = new clientSearchRequest_1.ClientSearchRequest();
    }
    ClientListComponent.prototype.ngOnInit = function () {
        this.search();
    };
    ClientListComponent.prototype.search = function (pageNumber) {
        var _this = this;
        if (pageNumber === void 0) { pageNumber = null; }
        if (pageNumber != null) {
            this.searchRequest.pageNumber = pageNumber;
        }
        this.searchResult = null;
        this.clientService.getClients(this.searchRequest).then(function (result) { return _this.searchResult = result; });
    };
    ClientListComponent.prototype.delayedSearch = function () {
        if (this.pendingSearchRequest != null) {
            clearTimeout(this.pendingSearchRequest);
        }
        var controller = this;
        this.pendingSearchRequest = setTimeout(function () {
            controller.search(1);
            controller.pendingSearchRequest = null;
        }, 750);
    };
    ClientListComponent.prototype.toggleAllClientsSelection = function (event) {
        for (var i in this.searchResult.clients) {
            this.searchResult.clients[i].selected = event.currentTarget.checked;
        }
    };
    ClientListComponent.prototype.getSelectedClients = function () {
        var clients = [];
        if (this.searchResult) {
            for (var i in this.searchResult.clients) {
                if (this.searchResult.clients[i].selected) {
                    clients.push(this.searchResult.clients[i]);
                }
            }
        }
        return clients;
    };
    ClientListComponent = __decorate([
        core_1.Component({
            selector: 'clientlist',
            templateUrl: './app/components/clientlist/clientlist.html',
            providers: [client_service_1.ClientService],
            pipes: [relativeTimePipe_1.RelativeTimePipe],
        }), 
        __metadata('design:paramtypes', [client_service_1.ClientService])
    ], ClientListComponent);
    return ClientListComponent;
}());
exports.ClientListComponent = ClientListComponent;
//# sourceMappingURL=clientlist.component.js.map