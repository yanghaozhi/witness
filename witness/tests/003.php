<?php

function abc($a)
{
    if ($a === null)
    {
        try{

            throw new exception("dddd");
        }catch(Exception $e)
        {
            // do nothing ...
        }
    }
    return $a + 10;
}

witness_start('test:123');

$b = abc(null);
print_r($b);
