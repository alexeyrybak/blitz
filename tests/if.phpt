--TEST--
predefined methods: if
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

$if_var_TRUE = 'yes, true';
$if_var_FALSE = 'yes, false';

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
TRUE yes, true
FALSE yes, false
FALSE yes, false
FALSE yes, false
FALSE yes, false
TRUE yes, true
TRUE yes, true
TRUE yes, true
FALSE yes, false
