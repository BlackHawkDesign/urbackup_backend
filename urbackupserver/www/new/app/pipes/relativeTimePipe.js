var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
///<reference path='../../typings/modules/moment/index.d.ts'/>
var core_1 = require('@angular/core');
let RelativeTimePipe = class {
    transform(value, exponent) {
        //return moment(value).fromNow();
        return "blabla";
    }
};
RelativeTimePipe = __decorate([
    core_1.Pipe({
        name: 'relativeTime'
    })
], RelativeTimePipe);
exports.RelativeTimePipe = RelativeTimePipe;
//# sourceMappingURL=relativeTimePipe.js.map