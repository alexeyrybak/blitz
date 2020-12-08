--TEST--
testing numbers
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{IF a == b}}a equals b{{ELSE}}a doesn't equals b{{END}}
{{IF c > d}}c is bigger then d{{ELSE}}c not bigger then d{{END}}
{{IF e >= f}}e is bigger or equal to f{{ELSE}}e not bigger or equal to f{{END}}
---

BODY;

$T = new Blitz();
$T->load($body);

// run 1, empty
$T->display(
	array(
	)
);

// run 2, null assign
$T->display(
	array(
		'a' => null,
		'b' => null,
		'c' => null,
		'd' => null,
		'e' => null,
		'f' => null
	)
);

// run 3, playtime
$T->display(
	array(
		'a' => 5,
		'b' => 5,
		'c' => 5,
		'd' => 5,
		'e' => 5,
		'f' => 5
	)
);

$T->display(
	array(
		'a' => 5.0,
		'b' => 5,
		'c' => 5.0,
		'd' => 5,
		'e' => 5.0,
		'f' => 5
	)
);

$T->display(
	array(
		'a' => 5.0,
		'b' => 5.0,
		'c' => 5.0,
		'd' => 5.0,
		'e' => 5.0,
		'f' => 5.0
	)
);

$T->display(
	array(
		'a' => "5",
		'b' => 5.0,
		'c' => "5.0",
		'd' => 5.0,
		'e' => "5.0",
		'f' => 5.0
	)
);

$T->display(
	array(
		'a' => "5",
		'b' => 5,
		'c' => "5",
		'd' => 5,
		'e' => "5",
		'f' => 5
	)
);

$T->display(
	array(
		'a' => "5ve",
		'b' => 5,
		'c' => "0 ducks",
		'd' => 0,
		'e' => "5.ve",
		'f' => 5.0
	)
);

?>
--EXPECT--
a doesn't equals b
c not bigger then d
e not bigger or equal to f
---
a equals b
c not bigger then d
e is bigger or equal to f
---
a equals b
c not bigger then d
e is bigger or equal to f
---
a equals b
c not bigger then d
e is bigger or equal to f
---
a equals b
c not bigger then d
e is bigger or equal to f
---
a equals b
c not bigger then d
e is bigger or equal to f
---
a equals b
c not bigger then d
e is bigger or equal to f
---
a doesn't equals b
c is bigger then d
e is bigger or equal to f
---
