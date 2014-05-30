<?php

$version = 0;
$data = file_get_contents('/Users/fisher/blitz/release/last/blitz/blitz.c');
if (preg_match("/#define BLITZ_VERSION_STRING \"([\.0-9]+)\"/", $data, $matches)) {
    $version = $matches[1];
}

echo $version;

?>
