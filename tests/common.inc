<?php

if (!class_exists('blitz'))
    dl('blitz.so');

$pwd = dirname(__FILE__)."/";
ini_set('blitz.path', $pwd);

ini_set('blitz.remove_spaces_around_context_tags', 0);

// set this handler to simplify error checking for both PHP4 and PHP5
error_reporting(E_ALL);
set_error_handler('default_error_handler');
function default_error_handler($errno, $errstr, $errfile, $errline) {
    $parts = split('ERROR:',$errstr);
    echo trim($parts[1])."\n";
}

?>
