"use strict";
var client_1 = require('./../models/client');
var clientStatus_1 = require('./../models/clientStatus');
var process_1 = require('./../models/process');
var clientSearchResult_1 = require('./../models/clientSearchResult');
var ClientService = (function () {
    function ClientService() {
        var client1 = new client_1.Client("Pc1");
        client1.id = 1;
        client1.fileOk = true;
        client1.imageOk = true;
        client1.lastFileBackup = new Date("2016-03-01");
        client1.lastFileBackup = new Date("2016-03-02");
        client1.lastSeen = new Date();
        client1.online = true;
        client1.osVersion = "Windows 7 64 biit";
        client1.processes = [new process_1.Process("0", 20)];
        client1.status = clientStatus_1.ClientStatus.InProgress;
        client1.ip = "192.168.1.2";
        client1.version = "1.5.1";
        var client2 = new client_1.Client("Pc2");
        client2.id = 2;
        client2.fileOk = true;
        client2.imageOk = true;
        client2.lastFileBackup = new Date("2016-03-01");
        client2.lastFileBackup = new Date("2016-03-02");
        client2.lastSeen = new Date();
        client2.online = true;
        client2.osVersion = "Windows 7 64 biit";
        client2.status = 0;
        this.clients = [
            client1,
            client2
        ];
        for (var i = 0; i < 100; i++) {
            var client = new client_1.Client("Pc " + i);
            client.id = 2;
            client.fileOk = true;
            client.imageOk = true;
            client.lastFileBackup = new Date("2016-03-01");
            client.lastFileBackup = new Date("2016-03-02");
            client.lastSeen = new Date("2016-04-02");
            client.online = false;
            client.osVersion = "Windows 7 64 biit";
            client.status = 0;
            client.ip = "192.168.1." + i;
            client.version = "1.5.1";
            client.status = clientStatus_1.ClientStatus.Outdated;
            this.clients.push(client);
        }
    }
    ClientService.prototype.getClients = function (searchRequest) {
        var clients = [];
        for (var i in this.clients) {
            var client = this.clients[i];
            if (client.name.toUpperCase().search(searchRequest.name.toUpperCase()) === -1) {
                continue;
            }
            if (searchRequest.online !== -1 && client.online !== (searchRequest.online === 1)) {
                continue;
            }
            if (searchRequest.status !== -1 && client.status !== searchRequest.status) {
                continue;
            }
            clients.push(this.clients[i]);
        }
        var start = (searchRequest.pageNumber - 1) * searchRequest.pageSize;
        var end = searchRequest.pageNumber * searchRequest.pageSize;
        return new Promise(function (resolve) {
            return setTimeout(function () { return resolve(new clientSearchResult_1.ClientSearchResult(searchRequest, clients.slice(start, end), clients.length)); }, 2000);
        });
    };
    return ClientService;
}());
exports.ClientService = ClientService;
//# sourceMappingURL=client.service.js.map