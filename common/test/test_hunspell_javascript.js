var fs = require("fs");
var Spellchecker = require("hunspell-spellchecker");

var spellchecker = new Spellchecker();

// Parse an hunspell dictionary that can be serialized as JSON
var DICT = spellchecker.parse({
    aff: fs.readFileSync("./hsb_DE_soblex_w8_3.09.11.aff"),
    dic: fs.readFileSync("./hsb_DE_soblex_w8_3.09.11.dic")
});

// Load a dictionary
spellchecker.use(DICT);

// Check a word
var isRight = spellchecker.check("jedyn");
console.log("jedyn: " + isRight)

var isRight = spellchecker.check("dwaj");
console.log("dwaj: " + isRight)

var isRight = spellchecker.check("ławka");
console.log("ławka: " + isRight)

var isRight = spellchecker.check("překłapjenje");
console.log("překłapjenje: " + isRight)

var isRight = spellchecker.check("překwapjenje");
console.log("překwapjenje: " + isRight)

