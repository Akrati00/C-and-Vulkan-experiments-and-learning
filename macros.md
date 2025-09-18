Socho tum ek factory mein ho jahan machine parts fit kar rahe ho.

Without macro: har baar tum khud part lagao → fir khud check karo sahi fit hua ya nahi.

With macro (VK_CHECK): ek robot tumhare sath khada hai jo har part lagate hi check kar leta hai. Agar galat hua toh line turant stop ho jaati hai → taaki baad mein pura machine kharab na ho.

Why it matters in Vulkan

Vulkan low-level API hai → mistakes accumulate karte ho to debugging hell ho jaata hai.
VK_CHECK tumhe turant crash + error message de deta hai → easy to fix problem early.

1. VK_CHECK
#define VK_CHECK(call)                                                         
  do {                                                                         
    VkResult result_ = call;                                                   
    assert(result_ == VK_SUCCESS);                                             
  } while (0)


👉 Matlab:

Jab bhi tum koi Vulkan function call karogi (jo VkResult return karta hai), ye macro check karega ki return value VK_SUCCESS hai ya nahi.

Agar success nahi hua, toh program assertion fail karke crash ho jaayega (aur tumhe turant pata chal jaayega ki error hua hai).

Example:

VK_CHECK(vkCreateInstance(&createInfo, NULL, &instance));


Ye expand hoke ban jaata hai:

do {
    VkResult result_ = vkCreateInstance(&createInfo, NULL, &instance);
    assert(result_ == VK_SUCCESS);
} while (0);


Tumhe manually likhna nahi pada, macro ne kaam asaan kar diya ✅
