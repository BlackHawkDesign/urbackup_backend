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
var NumberBinderDirective = (function () {
    function NumberBinderDirective(el) {
    }
    NumberBinderDirective.prototype.link = function (scope, element, attrs, ngModel) {
        /*
        ngModel.$parsers.push(function (val) {
            return val ? parseInt(val, 10) : null;
        });
        ngModel.$formatters.push(function (val) {
            return val ? '' + val : null;
        });*/
        alert("test!");
    };
    NumberBinderDirective = __decorate([
        core_1.Directive({ selector: '[numberBinder]' }), 
        __metadata('design:paramtypes', [Object])
    ], NumberBinderDirective);
    return NumberBinderDirective;
}());
exports.NumberBinderDirective = NumberBinderDirective;
//# sourceMappingURL=numberBinder.directive.js.map