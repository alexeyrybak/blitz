--TEST--
complex conditions 4
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{IF object.a && object.b != '1' }}
a and b != 1
{{ELSE}}
condition not met
{{END}}
###

BODY;

$obj = new stdClass();
$obj->a = true;
$obj->b = 2;

$T = new Blitz();
$T->load($body);
$T->display(
    array(
        'object' => $obj
    )
);
$obj->b = 1;
$T->display(
    array(
        'object' => $obj
    )
);
$obj->b = null;
$T->display(
    array(
        'object' => $obj
    )
);

?>
--EXPECT--
a and b != 1
###
condition not met
###
a and b != 1
###
