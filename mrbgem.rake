MRuby::Gem::Specification.new('mruby-curl') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'

  spec.add_dependency 'mruby-http'

  spec.linker.libraries << 'curl'
  spec.linker.libraries << 'ssl'
  spec.linker.libraries << 'crypto'
  spec.linker.libraries << 'z'
  require 'open3'

  curl_version  = "7.50.2"
  curl_url      = "https://curl.haxx.se/download"
  curl_package  = "curl-#{curl_version}.tar.gz"

  curl_dir      = "#{build_dir}/curl-#{curl_version}"

  def run_command env, command
    STDOUT.sync = true
    puts "build: [exec] #{command}"
    Open3.popen2e(env, command) do |stdin, stdout, thread|
      print stdout.read
      fail "#{command} failed" if thread.value != 0
    end
  end

  FileUtils.mkdir_p build_dir

  if ! File.exists? curl_dir
    Dir.chdir(build_dir) do
      e = {}
      run_command e, "curl #{curl_url}/#{curl_package} | tar -xzv"
      run_command e, "mkdir #{curl_dir}/build"
    end
  end

  if ! File.exists? "#{curl_dir}/build/lib/libcurl.a"
    Dir.chdir curl_dir do
      e = {
        'CC' => "#{spec.build.cc.command} #{spec.build.cc.flags.join(' ')}",
        'CXX' => "#{spec.build.cxx.command} #{spec.build.cxx.flags.join(' ')}",
        'LD' => "#{spec.build.linker.command} #{spec.build.linker.flags.join(' ')}",
        'AR' => spec.build.archiver.command,
        'PREFIX' => "#{curl_dir}/build"
      }

      configure_opts = %w(--prefix=$PREFIX --enable-static --disable-shared --disable-ldap)
      if build.kind_of?(MRuby::CrossBuild) && build.host_target && build.build_target
        configure_opts += %W(--host #{spec.build.host_target} --build #{spec.build.build_target})
      end
      run_command e, "./configure #{configure_opts.join(" ")}"
      run_command e, "make"
      run_command e, "make install"
    end
  end

  spec.cc.include_paths << "#{curl_dir}/build/include"
  spec.linker.library_paths << "#{curl_dir}/build/lib/"
end
