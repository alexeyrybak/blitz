--TEST--
comments
--FILE--
<?php

ini_set('blitz.enable_comments', 1);

$T = new Blitz();

$body = 'hey! /* gonna begin */ you! {{ BEGIN a }} get /* some comment */ off /* some comment again */ my /* come get some! */ cloud {{ END }} ! /* the end */'; 

$T->load($body);

$data = array(
    'a' => array(array())
);

$T->display($data);

?>
--EXPECT--
hey!  you!  get  off  my  cloud  ! 
