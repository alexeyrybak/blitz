--TEST--
mixed set
--FILE--
<?php
include('common.inc');

$tpl = <<<E_TPL
<!-- BEGIN a -->a_var = {{ \$a_var }}
<!-- BEGIN b -->b_var = {{ \$b_var }}
<!-- BEGIN c -->c_var = {{ \$c_var }}<!-- END c -->
<!-- END b -->
<!-- END a -->
E_TPL;

$T = new Blitz;
$T->load($tpl);

$set['a'] = array('a_var' => 'a-value');
$set['a']['b']['c']['c_var'] = 'c-value';
$T->set($set);

echo $T->parse();

?>
--EXPECT--
a_var = a-value
b_var = 
c_var = c-value

