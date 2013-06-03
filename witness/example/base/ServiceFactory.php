<?php

class ServiceFactory
{
	private static $_objCache = array();

	public static function getService($name)
	{
		if(!isset($_objCache[$name]))
		{
			$_objCache[$name] = new $name();
		}
		return $_objCache[$name];
	}
}

