"use strict";
(function (ClientStatus) {
    ClientStatus[ClientStatus["Ok"] = 0] = "Ok";
    ClientStatus[ClientStatus["Outdated"] = 1] = "Outdated";
    ClientStatus[ClientStatus["InProgress"] = 2] = "InProgress";
})(exports.ClientStatus || (exports.ClientStatus = {}));
var ClientStatus = exports.ClientStatus;
//# sourceMappingURL=clientStatus.js.map