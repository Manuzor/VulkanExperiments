
VertexShader {
  uniform "GlobalsBuffer" Binding=0 {
    mat4 "ViewProjectionMatrix"
  }

  uniform "ModelBuffer" Binding=1 {
    mat4 "ModelViewProjectionMatrix"
  }

  Input {
    vec3 "VertexPosition" Location=0
    vec2 "VertexUV" Location=1
  }

  Output {
    vec2 "UV" Location=0
  }

  Code Entry="main" {
    `
    void main()
    {
      gl_Position = ModelViewProjectionMatrix * vec4(VertexPosition, 1.0f);
      UV = VertexUV;
    }
    `
  }
}

FragmentShader {
  sampler2D "Sampler" Binding=10

  Input {
    vec2 "UV" Location=0
  }

  Output {
    vec4 "FragmentColor" Location=0
  }

  Code Entry="main" {
    `
    void main()
    {
      FragmentColor = texture(Sampler, UV);
    }
    `
  }
}
