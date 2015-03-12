--TEST--
testing not operator
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{IF !a}}not a{{ELSE}}a{{END}}
{{IF !!a}}not not a{{ELSE}}not a{{END}}
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
		'a' => null
	)
);

// run 3, boolean
$T->display(
	array(
		'a' => true
	)
);

$T->display(
	array(
		'a' => false
	)
);

// run 4, strings / doubles / everything else
$T->display(
	array(
		'a' => ""
	)
);

$T->display(
	array(
		'a' => "not empty"
	)
);

$T->display(
	array(
		'a' => "0 ducks"
	)
);

$T->display(
	array(
		'a' => "1 rabbit"
	)
);

$T->display(
	array(
		'a' => 0.0
	)
);

$T->display(
	array(
		'a' => 0.2
	)
);

?>
--EXPECT--
not a
not a
---
not a
not a
---
a
not not a
---
not a
not a
---
not a
not a
---
a
not not a
---
a
not not a
---
a
not not a
---
not a
not a
---
a
not not a
---
