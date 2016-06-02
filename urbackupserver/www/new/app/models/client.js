"use strict";
var Client = (function () {
    function Client(name) {
        this.name = name;
        this.selected = false;
        this.showDetail = false;
        this.status = ClientStatus.Ok;
        this.processes = [];
    }
    Client.prototype.isCreatingBackups = function () {
        return this.processes.length > 0;
    };
    Client.prototype.getTotalProcessPercentage = function () {
        var totalPercentange = 0;
        for (var i in this.processes) {
            totalPercentange += this.processes[i].percentage;
        }
        return this.processes.length === 0 ? totalPercentange : totalPercentange / this.processes.length;
    };
    return Client;
}());
exports.Client = Client;
//# sourceMappingURL=client.js.map