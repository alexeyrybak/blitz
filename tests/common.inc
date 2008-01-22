<?php

$pwd = dirname(__FILE__)."/";
ini_set('blitz.path', $pwd);
if (!function_exists('blitz'))
    dl('blitz.so');

?>
