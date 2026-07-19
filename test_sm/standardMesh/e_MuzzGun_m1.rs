subshader "e_MuzzGun_m1_Material0" "StandardMesh/Default"
{
	lighting false;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	transparent true;
	blendSrc sourceAlpha; 
	blendDest one;
	twosided true;
	depthWrite false;
	texture "texture/e_MuzzGun_o";
}

