System.register(['./../models/client', './../models/clientSearchResult'], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var client_1, clientSearchResult_1;
    var ClientService;
    return {
        setters:[
            function (client_1_1) {
                client_1 = client_1_1;
            },
            function (clientSearchResult_1_1) {
                clientSearchResult_1 = clientSearchResult_1_1;
            }],
        execute: function() {
            ClientService = (function () {
                /*
                id: number;
              name: string;
              fileOk: boolean;
              imageOk: boolean;
              lastFileBackup: Date;
              lastImageBackup: Date;
              lastSeen: Date;
              Online: boolean;
              OsVersion: string;
              selected: boolean;
              Status: number;*/
                function ClientService() {
                    var client = new client_1.Client("Pc1");
                    client.id = 1;
                    client.fileOk = true;
                    client.imageOk = true;
                    client.lastFileBackup = new Date("2016-03-01");
                    client.lastFileBackup = new Date("2016-03-02");
                    client.lastSeen = new Date();
                    client.online = true;
                    client.osVersion = "Windows 7 64 biit";
                    client.status = 0;
                    var client2 = new client_1.Client("Pc1");
                    client2.id = 1;
                    client2.fileOk = true;
                    client2.imageOk = true;
                    client2.lastFileBackup = new Date("2016-03-01");
                    client2.lastFileBackup = new Date("2016-03-02");
                    client2.lastSeen = new Date();
                    client2.online = false;
                    client2.osVersion = "Windows 7 64 biit";
                    client2.status = 0;
                    this.clients = [
                        client,
                        client2
                    ];
                }
                ClientService.prototype.getClients = function (searchRequest) {
                    var clients = [];
                    for (var i in this.clients) {
                        if (this.clients[i].name.toUpperCase().search(searchRequest.name.toUpperCase()) != -1) {
                            clients.push(this.clients[i]);
                        }
                    }
                    return new clientSearchResult_1.ClientSearchResult(searchRequest, clients);
                };
                return ClientService;
            }());
            exports_1("ClientService", ClientService);
        }
    }
});
//# sourceMappingURL=client.service.js.map