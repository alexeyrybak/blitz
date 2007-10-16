--TEST--
method call from inner include
--FILE---
<?php
include('common.inc');

class View extends Blitz {
    function test($p1, $p2 = 'empty') {
        return 'View::test call, p1 = "'.$p1.'", p2="'.$p2."\"\n";
    }
}

$T = new View('mfi.tpl');

$T->set_global(array('var' => "hello, world!"));
echo $T->parse();

?>
--EXPECT--
Method from include dest
test: View::test call, p1 = "some", p2="hello, world!"
