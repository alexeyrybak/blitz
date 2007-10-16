--TEST--
variables
--FILE--
<?php
include('common.inc');

$T = new Blitz('var.tpl');

$T->set(
    array(
        'a' => 'a_value', 
        'b' => 'b_value',
        'c' => 'c_value'
    )
);

echo $T->parse();

?>
--EXPECT--
a_value
b_value
c_value
