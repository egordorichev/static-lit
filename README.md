### lit

lit is a tiny and fast language, designed to be embeddable and used as a scripting language. It takes inspiration from JavaScript, Java, C# and Lua, and combines them all into a soft-typed stack-vm based beast.

Here is how it looks:

```js
fun do_thing(string a) > int {
    print(a)
    return 3
}

var x = 10
var a = do_thing(a) // -> 10, a is 3
```

Please note, that the language is still in it's early stages, and I would recommend not using it right now. Tho it's growing pretty fast ;)

### Building

To build lit, you will only need cmake and make:

```
cd lit
cmake .
make
```

To run a program simply use:

```
./lit main.lit
```

### Thanks

Big thanks to [Bob Nystrom](https://twitter.com/munificentbob) for his amazing [book about interpreters](http://craftinginterpreters.com/). Make sure to check it out!
