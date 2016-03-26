System.register(['angular2/core'], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
        var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
        else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = (this && this.__metadata) || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var core_1;
    var ClientListComponent;
    return {
        setters:[
            function (core_1_1) {
                core_1 = core_1_1;
            }],
        execute: function() {
            ClientListComponent = (function () {
                function ClientListComponent() {
                    this._clients = [{ "name": "Pc1" }, { "name": "PC2" }];
                    this.clients = this._clients;
                    this.filter = { name: "", online: "", status: "" };
                }
                ClientListComponent.prototype.executeFilter = function () {
                    this.clients = [];
                    for (var i in this._clients) {
                        if (this._clients[i].name.toUpperCase().search(this.filter.name.toUpperCase()) != -1) {
                            this.clients.push(this._clients[i]);
                        }
                    }
                };
                ClientListComponent.prototype.displayDetail = function (clientNameCell) {
                    var row = $(clientNameCell).parent();
                    var detailRow = row.next();
                    detailRow.show();
                };
                ClientListComponent = __decorate([
                    core_1.Component({
                        selector: 'clientlist',
                        templateUrl: './app/components/clientlist/clientlist.html'
                    }), 
                    __metadata('design:paramtypes', [])
                ], ClientListComponent);
                return ClientListComponent;
            }());
            exports_1("ClientListComponent", ClientListComponent);
        }
    }
});
//# sourceMappingURL=clientlist.component.js.map