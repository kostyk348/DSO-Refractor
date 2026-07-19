shader "Material0"
{
	technique
	{
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 0 0 0;
			materialSpecular 0 0 0;
			materialSpecularPower 12;
			alphaTest greater 0.7;
 			
			stage
			{
				texture "texture/M3APC_Whell1_Z";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

shader "Material1"
{
	technique
	{
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 0 0 0;
			materialSpecular 0 0 0;
			materialSpecularPower 12;
			alphaTest greater 0.7;

			stage
			{
				texture "texture/M3APC_Whell2_Z";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

