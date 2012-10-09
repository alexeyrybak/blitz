--TEST--
clean_globals
--FILE--
<?php
include('common.inc');
$body = <<<BODY
================================
globals cleaning
================================
{{ \$global }}
<!-- BEGIN a -->context (a)
    {{ \$global }};{{ \$local }}
<!-- END -->
BODY;

$T = new Blitz();
$T->load($body);
$T->setGlobals(array('global' => 'Global'));
echo $T->parse(array('a' => array('local' => 'Local')));

$T->clean();
echo $T->parse();

$T->cleanGlobals();
echo $T->parse();

$T->setGlobals(array('global' => 'NewGlobal'));
echo $T->parse();
?>
--EXPECT--
================================
globals cleaning
================================
Global
context (a)
    Global;Local
================================
globals cleaning
================================
Global
================================
globals cleaning
================================

================================
globals cleaning
================================
NewGlobal
