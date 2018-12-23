### lit

lit is a tiny and fast language, designed to be embeddable and used as a scripting language. It takes inspiration from JavaScript, Java, C# and Lua, and combines them all into a statically-typed stack-vm based language.

Here is how it looks:

```js
int do_thing(String a) {
	print(a)
	return 3
}

var x = 10
var a = do_thing("10") // -> 10, a is 3

class Awesome {
  var test = "Awesome" // Protected by default  
    
	void doStuff() { // Public by default
		print(this.test)
	}
}

var a = Awesome()
a.doStuff() // -> Awesome

if true { 
	// You can ignore () if you want, but you sill can
	// do this: if (true) { ... }
	// or this: if !(true && false) { .. }
	print("Truth is right!") 
}
```

Please note, that the language is still in it's early stages, and I would recommend not using it right now. Tho it's growing pretty fast ;)

### Building

To build lit, you will only need cmake, make and GCC (4.9.0+):

```
cd lit
cmake .
make
```

To run a program simply use:

```
./lit test/main.lit
```

### Thanks

Big thanks to [Bob Nystrom](https://twitter.com/munificentbob) for his amazing [book about interpreters](http://craftinginterpreters.com/). Make sure to check it out!
