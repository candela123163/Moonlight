#pragma once


template<typename T>
class ResourceContainer
{
public:
    bool Contains(size_t key) const
    {
        return mResource.find(key) != mResource.end();
    }

    T* Get(size_t key)
    {
        auto it = mResource.find(key);
        return it != mResource.end() ? it->second.get() : nullptr;
    }

    T* Insert(size_t key, std::unique_ptr<T>&& resource)
    {
        auto reslut = mResource.insert({ key, std::move(resource) });
        return reslut.second ? reslut.first->second.get() : nullptr;
    }

    size_t Size() const
    {
        return mResource.size();
    }

private:
    std::unordered_map<size_t, std::unique_ptr<T>> mResource;
};

