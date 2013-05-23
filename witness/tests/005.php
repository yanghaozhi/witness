<?php

class c1
{
    static public function a($a, $aa)
    {
        $c = array($aa => $a, 'abcd' => $aa + $a);
        witness_dump('abcdefg');
        $d = $aa - $a;
        return $c;
    }
};

function f1($a, $b)
{
    $d = c1::a(5, $a + $b);
    $a = $a + $d['abcd'];
    echo f2($a + $b, $a * $b) . "\n";
}

function f2($a, $b)
{
    echo ($a * $b) . "\n";
    return json_encode(c1::a($a, $b));
}

function ff()
{
    witness_start('abc');
    return 1;
}

echo "abc\n";

ff();

echo f1(5, 6) . "\n";
echo "123\n";
f1(5, 10);
