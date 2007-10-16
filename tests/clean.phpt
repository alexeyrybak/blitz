--TEST--
clean
--FILE--
<?php
include('common.inc');
$body = <<<BODY
================================
context cleaning ({{ \$var }})
================================
<!-- BEGIN a -->context (a)
    {{ \$va }}
    <!-- BEGIN b -->context (b)
        {{ \$vb }}
    <!-- END b -->
<!-- END -->
BODY;

$T = new Blitz();
$T->load($body);
$T->set(array('var' => 'Hello, World!'));
$T->block('/a', array('va' => 'va_value'));
for($i=0; $i<3; $i++) {
    $T->block('/a/b', array('vb' => 'vb_value_'.$i));
}
echo $T->parse();

$T->clean('/a/b');
echo $T->parse();

$T->clean('/a');
echo $T->parse();

$T->block('/a', array('va' => 'va_value_new'));
$T->iterate('/a/b');
echo $T->parse();

$T->clean();
echo $T->parse();
$T->clean('/',FALSE); // clean with no "not found" warning

?>
--EXPECT--
================================
context cleaning (Hello, World!)
================================
context (a)
    va_value
    context (b)
        vb_value_0
    context (b)
        vb_value_1
    context (b)
        vb_value_2
    
================================
context cleaning (Hello, World!)
================================
context (a)
    va_value
    
================================
context cleaning (Hello, World!)
================================
================================
context cleaning (Hello, World!)
================================
context (a)
    va_value_new
    context (b)
        
    
================================
context cleaning ()
================================
