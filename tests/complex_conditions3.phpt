--TEST--
complex conditions 3
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{IF array && boolean }}
Array and boolean
{{ELSE}}
Not array or not boolean
{{END}}
{{IF array2 && !boolean2 }}
Array2 and not boolean2
{{ELSEIF !array2 && boolean}}
Not array2 but boolean2
{{ELSE}}
Not array2 or boolean2
{{END}}
{{IF array}}
Array
{{ELSE}}
Not array
{{END}}
{{IF boolean}}
Boolean
{{ELSE}}
Not boolean
{{END}}
BODY;

$T = new Blitz();
$T->load($body);
$T->display(
    array(
        'array' => array('a' => 'b'),
        'boolean' => true,
        'array2' => array('a' => 'b'),
        'boolean2' => true
    )
);
$T->display(
    array(
        'array' => array(),
        'boolean' => true,
        'array2' => array(),
        'boolean2' => true
    )
);
$T->display(
    array(
        'array' => array('a' => 'b'),
        'boolean' => false,
        'array2' => array('a' => 'b'),
        'boolean2' => false
    )
);
$T->display(
    array(
        'array' => array(),
        'boolean' => false,
        'array2' => array(),
        'boolean2' => false
    )
);

?>
--EXPECT--
Array and boolean
Not array2 or boolean2
Array
Boolean
Not array or not boolean
Not array2 but boolean2
Not array
Boolean
Not array or not boolean
Array2 and not boolean2
Array
Not boolean
Not array or not boolean
Not array2 or boolean2
Not array
Not boolean
