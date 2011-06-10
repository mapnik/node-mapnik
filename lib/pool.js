/* obj pool to manage concurrency */

try { var pool = require('generic-pool').Pool; } catch (e) {};

create = function(size) {
    if (!pool) {
        throw new Error("map pool requires generic-pool to be installed");
    }
    return {
        max: size || 5,
        pools: {},
        acquire: function(id, options, callback) {
            if (!this.pools[id]) {
                var that = this;
                this.pools[id] = pool({
                    name: id,
                    create: options.create,
                    destroy: options.destroy,
                    max: that.max,
                    idleTimeoutMillis: options.idleTimeoutMillis || 5000,
                    log: false
                    //reapIntervalMillis
                    //priorityRange
                });
            }
            this.pools[id].acquire(callback, options.priority);
        },
        release: function(id, obj) {
            this.pools[id] && this.pools[id].release(obj);
        }
    };
};

exports.create = create;

