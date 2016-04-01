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
                }
                return Client;
            }());
            exports_1("Client", Client);
        }
    }
});
/*

{
         "client_version_string":"",
         "delete_pending":"",
         "file_ok":false,
         "id":1,
         "image_ok":false,
         "ip":"-",
         "lastbackup":"",
         "lastbackup_image":"",
         "lastseen":"2016-03-31 20:35",
         "name":"test1",
         "online":false,
         "os_version_string":"",
         "processes":[

         ],
         "status":0
      },
      */ 
//# sourceMappingURL=client.js.map