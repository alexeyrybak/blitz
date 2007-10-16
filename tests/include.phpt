--TEST--
predefined methods: include
--FILE--
<?php
include('common.inc');

$T = new Blitz('include.tpl');

$T->set_global(
    array(
        'a' => 'a_value',
        'b' => 'b_value',
        'c' => 'c_value'
    )
);

echo $T->parse();

?>
--EXPECT--
include test: a = a_value, b = b_value

