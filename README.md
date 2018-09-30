# lit

lit is a tiny and fast language, designed to be embeddable and used as a scripting language. It takes inspiration from JavaScript, Java, C# and Lua, and combines them all into a soft-typed stack-vm based beast.

Here is how it looks:

```js
class Test {
    do_stuff() {
        print("Hi!")
    }
}

var test = Test()
test.do_stuff() // -> Hi!

fun do_thing(a) {
    print(a)
    return 3, "hi"
}

var x = 10
var a, b = do_thing(a) // -> 10, a and b are now 3 and "hi"
```

Please note, that the language is still in it's early stages, and I would recommend not using it right now. Tho it's growing pretty fast ;)

# Thanks

Big thanks to [Bob Nystrom](https://twitter.com/munificentbob) for his amazing [book about interpreters](http://craftinginterpreters.com/). Make sure to check it out!
