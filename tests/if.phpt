--TEST--
if/unless predefined methods
--FILE--
<?php
include('common.inc');

$test_vars = array(
    TRUE,
    FALSE,
    NULL,
    0,
    "0",
    1,
    "hello, world!",
    1976
);

$if_var_TRUE = 'true_var';
$if_var_FALSE = 'false_val';

$T = new Blitz('if.tpl');
$T->set_global(
    array(
        'if_var_TRUE' => $if_var_TRUE,
        'if_var_FALSE' => $if_var_FALSE
    )
);

foreach($test_vars as $var) {
    $T->set(array('var' => $var));
    echo $T->parse();
}

$T->clean();
echo $T->parse();

?>
--EXPECT--
if: TRUE true_var
unless: FALSE false_val
if: FALSE false_val
unless: TRUE true_var
if: FALSE false_val
unless: TRUE true_var
if: FALSE false_val
unless: TRUE true_var
if: FALSE false_val
unless: TRUE true_var
if: TRUE true_var
unless: FALSE false_val
if: TRUE true_var
unless: FALSE false_val
if: TRUE true_var
unless: FALSE false_val
if: FALSE false_val
unless: TRUE true_var
