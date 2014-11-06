## Logger

#### Constructor

- No constructor - Severity level is only available via `mapnik.Logger` static instance

#### Methods

- `getSeverity()` : Returns integer which represents severity level
- `setSeverity(severity)` : Accepts level of severity as a mapnik constant

#### Available severity levels (and the int they represent)
- mapnik.Logger.DEBUG -> 0
- mapnik.Logger.WARN -> 1
- mapnik.Logger.ERROR (default) -> 2
- mapnik.Logger.NONE -> 3


#### Example
```
mapnik.Logger.setSeverity(mapnik.Logger.NONE);

var logging = getSeverity();    //logging should equal 3
```