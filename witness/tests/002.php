<?php

function abc($aaaaaaaaaa, $bbbbbbbbbb)
{
    $cccccccccc = $aaaaaaaaaa + $bbbbbbbbbb;
    $dddddddddd = $cccccccccc * $cccccccccc;
    witness_core_dump($cccccccccc + $dddddddddd);
    return ($dddddddddd - $cccccccccc);
}

function abcd($aaaaa, $bbbbb)
{
    $ccccc = $aaaaa - $bbbbb;
    return abc($ccccc, $aaaaa + $bbbbb);
}

define('test', 12345);
$qwer = 'abcdefg';
//witness_start("abc");

echo abcd(10, 20) . "\n";
