--TEST--
mix #4
--FILE--
<?php
include('common.inc');

$T = new Blitz();

$body = <<<BODY
hello, {{ \$world }}
<!-- BEGIN a -->a: {{ \$a }}<!-- END a -->
{{ BEGIN b }}b: {{ \$b }}{{ END }}
<!-- BEGIN c -->c: {{ \$c }}<!-- END c -->
BODY;

$T->load($body);

for($i=0; $i<1000; $i++) {
    foreach(array('a','b','c') as $ctx_name) {
        $T->context('/'.$ctx_name);
        $T->set(array($ctx_name => $ctx_name.'_val_#'.$i));
    }
}

$T->context('/');
$T->set(array('world' => 'World!'));

echo $T->parse();

?>
--EXPECT--
hello, World!
a: a_val_#999
b: b_val_#999
c: c_val_#999
