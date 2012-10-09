--TEST--
custom "long" comment tags
--FILE--
<?php

include('common.inc');

ini_set('blitz.enable_comments', 1);
ini_set('blitz.enable_alternative_tags', 0);
ini_set('blitz.comment_open', '<!-- /*');
ini_set('blitz.comment_close', '*/ -->');

$T = new Blitz();

$body = '<!-- normal html comment --> then hey! <!-- /* gonna begin */ --> you! {{ BEGIN a }} get <!-- /* some comment */ --> off <!-- /* some comment again */ --> my <!-- /* come get some! */ --> cloud {{ END }} ! <!-- /* the end */ -->'; 

$T->load($body);

$data = array(
    'a' => array(array())
);

$T->display($data);

?>
--EXPECT--
<!-- normal html comment --> then hey!  you!  get  off  my  cloud  !
