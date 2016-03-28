System.register(['./../models/client'], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var client_1;
    var ClientService;
    return {
        setters:[
            function (client_1_1) {
                client_1 = client_1_1;
            }],
        execute: function() {
            ClientService = (function () {
                function ClientService() {
                    this.clients = [
                        new client_1.Client("Pc1"),
                        new client_1.Client("PC2")
                    ];
                }
                ClientService.prototype.getClients = function (searchRequest) {
                    var clients = [];
                    for (var i in this.clients) {
                        if (this.clients[i].name.toUpperCase().search(searchRequest.name.toUpperCase()) != -1) {
                            clients.push(this.clients[i]);
                        }
                    }
                    return clients;
                };
                return ClientService;
            }());
            exports_1("ClientService", ClientService);
        }
    }
});
//# sourceMappingURL=client.service.js.map