--TEST--
Bug #71 (segfault on clean() and fetch())
--FILE--
<?php

for ($i = 0; $i < 10; ++ $i) {
    $t = new \blitz();
    $t->load('');
    $t->set(['_BODY' => []]);
    $t->clean('/_BODY');
}
// and the same with fetch()!
for ($i = 0; $i < 10; ++ $i) {
    $t = new \blitz();
    $t->load('');
    $t->set(['_BODY' => []]);
    $t->fetch('/_BODY');
}
var_dump("you supposed to see me, not segfault");
?>
--EXPECTF--
Warning: blitz::fetch(): cannot find context /_BODY in template  in %s on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d

Warning: blitz::fetch(): cannot find context /_BODY in template  in %s/bug71_segfault.php on line %d
string(36) "you supposed to see me, not segfault"