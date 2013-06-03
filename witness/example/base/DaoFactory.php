<?php

class DaoFactory
{
	private static $_objCache = array();

	public static function getDao($name)
	{
		$name .= "Dao";
		if(!isset($_objCache[$name]))
		{
			$_objCache[$name] = new $name();
		}
		return $_objCache[$name];
	}
}

