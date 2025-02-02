#pragma once
#include <Mlib/Io/IIStream_Dictionary.hpp>
#include <Mlib/Map/Map.hpp>
#include <Mlib/Memory/Dangling_Base_Class.hpp>
#include <memory>
#include <mutex>
#include <string>

namespace Mlib {

struct StreamSegment;

template <class TStreamOwner>
class IStreamAndLock;

class ImgReader: public IIStreamDictionary, public virtual DanglingBaseClass {
    ImgReader(const ImgReader&) = delete;
    ImgReader& operator = (const ImgReader& other) = delete;
    friend IStreamAndLock<DanglingBaseClassRef<ImgReader>>;
public:
    ImgReader(std::istream& directory, std::unique_ptr<std::istream>&& data);
    static std::shared_ptr<IIStreamDictionary> load_from_file(const std::string& img_filename);
    virtual ~ImgReader() override;
    virtual std::vector<std::string> names() const override;
    virtual StreamAndSize read(
        const std::string& name,
        std::ios::openmode openmode,
        SourceLocation loc) override;
private:
    Map<std::string, StreamSegment> directory_;
    std::unique_ptr<std::istream> data_;
    std::recursive_mutex mutex_;
    bool reading_;
};

}
