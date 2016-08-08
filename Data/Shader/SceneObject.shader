
VertexShader {
  uniform "Globals" Binding=0 {
    mat4 "ViewProjectionMatrix"
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
      gl_Position = ViewProjectionMatrix * vec4(VertexPosition, 1.0f);
      UV = VertexUV;
    }
    `
  }
}

FragmentShader {
  sampler2D "Sampler" Binding=1

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
