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
var router_1 = require('@angular/router');
var menu_component_1 = require('./../menu/menu.component');
var clientlist_component_1 = require('./../clientlist/clientlist.component');
var logtable_component_1 = require('./../logtable/logtable.component');
var AppComponent = (function () {
    function AppComponent() {
    }
    AppComponent = __decorate([
        core_1.Component({
            selector: 'urbackup-app',
            templateUrl: './app/components/app/app.html',
            directives: [menu_component_1.MenuComponent, clientlist_component_1.ClientListComponent, router_1.ROUTER_DIRECTIVES]
        }),
        router_1.Routes([
            { path: '/', component: clientlist_component_1.ClientListComponent },
            { path: '/clients', component: clientlist_component_1.ClientListComponent },
            { path: '/logs', component: logtable_component_1.LogTableComponent }
        ]), 
        __metadata('design:paramtypes', [])
    ], AppComponent);
    return AppComponent;
}());
exports.AppComponent = AppComponent;
//# sourceMappingURL=app.component.js.map