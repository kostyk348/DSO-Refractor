subshader "1p_Tank_GunnerS_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.117647 0.117647 0.117647;
	materialSpecularPower 12.5;
	texture "texture/1P_Tanks_GunnerS_I";
}

subshader "1p_Tank_GunnerS_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	transparent true;
	envmap true;
	texture "texture/1P_Tanks_GunnerS_I";
}

