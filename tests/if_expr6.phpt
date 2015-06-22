--TEST--
testing or operator
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{IF telephone || mobile}}we have them{{ELSE}}we don't have them{{END}}

{{IF a || b}}a or b{{ELSE}}NOT a or b{{END}}
----

BODY;

$T = new Blitz();
$T->load($body);

$T->display(
	array(
		'telephone' => "+31",
		'mobile' => "+32",
		'a' => true,
		'b' => true
	)
);


$T->clean();
$T->display(
	array(
		'mobile' => "+33",
		'a' => false,
		'b' => true
	)
);

$T->clean();
$T->display(
	array(
		'telephone' => "+34",
		'a' => true,
		'b' => false
	)
);

$T->clean();
$T->display(
	array(
		'a' => false,
		'b' => false
	)
);

$T->clean();
$T->display(
	array(
		'telephone' => "+35",
		'a' => true,
		'b' => null
	)
);

?>
--EXPECT--
we have them

a or b
----
we have them

a or b
----
we have them

a or b
----
we don't have them

NOT a or b
----
we have them

a or b
----
