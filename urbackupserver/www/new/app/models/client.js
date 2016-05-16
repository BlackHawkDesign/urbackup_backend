System.register([], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var Client;
    return {
        setters:[],
        execute: function() {
            Client = (function () {
                function Client(name) {
                    this.name = name;
                    this.selected = false;
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
            exports_1("Client", Client);
        }
    }
});
//# sourceMappingURL=client.js.map